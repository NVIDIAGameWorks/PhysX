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



#ifndef APEX_ACTOR_H
#define APEX_ACTOR_H

#include "ApexContext.h"
#include "ApexRenderable.h"
#include "ApexResource.h"
#include "ResourceProviderIntl.h"
#include "ApexSDK.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxActor.h"
#include "PxShape.h"
#include "PxFiltering.h"
#include "PxRigidDynamic.h"
#include "PxTransform.h"
#include "PxRigidBodyExt.h"
#include "../../include/Actor.h"
#endif

#define UNIQUE_ACTOR_ID 1

namespace nvidia
{
namespace apex
{

class ApexContext;
class RenderDebugInterface;
class SceneIntl;
class Asset;
class Actor;

/**
Class that implements actor interface with its context(s)
*/
class ApexActor : public ApexRenderable, public ApexContext
{
public:
	ApexActor();
	~ApexActor();

	void				addSelfToContext(ApexContext& ctx, ApexActor* actorPtr = NULL);
	void				updateIndex(ApexContext& ctx, uint32_t index);
	bool				findSelfInContext(ApexContext& ctx);

	// Each class that derives from ApexActor should implement the following functions
	// if it wants ActorCreationNotification and Deletion callbacks
	virtual Asset*	getAsset(void)
	{
		return NULL;
	}
	virtual void			ContextActorCreationNotification(AuthObjTypeID	authorableObjectType,
	        ApexActor*		actorPtr)
	{
		PX_UNUSED(authorableObjectType);
		PX_UNUSED(actorPtr);
		return;
	}
	virtual void			ContextActorDeletionNotification(AuthObjTypeID	authorableObjectType,
	        ApexActor*		actorPtr)
	{
		PX_UNUSED(authorableObjectType);
		PX_UNUSED(actorPtr);
		return;
	}

	// Each class that derives from ApexActor may optionally implement these functions
	virtual Renderable* getRenderable()
	{
		return NULL;
	}
	virtual Actor*      getActor()
	{
		return NULL;
	}

	virtual void        release() = 0;
	void				destroy();

#if PX_PHYSICS_VERSION_MAJOR == 3
	virtual void		setPhysXScene(PxScene* s) = 0;
	virtual PxScene*	getPhysXScene() const = 0;
#endif

	enum ActorState
	{
		StateEnabled,
		StateDisabled,
		StateEnabling,
		StateDisabling,
	};

	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state)
	{
		mEnableDebugVisualization = state;
	}

protected:
	bool				mInRelease;

	struct ContextTrack
	{
		uint32_t	index;
		ApexContext*	ctx;
	};
	physx::Array<ContextTrack> mContexts;

#if UNIQUE_ACTOR_ID
	static int32_t mUniqueActorIdCounter;
	int32_t mUniqueActorId;
#endif

	bool				mEnableDebugVisualization;
	friend class ApexContext;
};



#if PX_PHYSICS_VERSION_MAJOR == 3

#define APEX_ACTOR_TEMPLATE_PARAM(_type, _name, _variable) \
bool set##_name(_type x) \
{ \
	_variable = x; \
	return is##_name##Valid(x); \
} \
_type get##_name() const { return _variable; }

#define APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(_name, _variable) \
if (!is##_name##Valid(_variable)) \
{ \
	return false; \
}

#define APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(_name)	set##_name( getDefault##_name() );

// template for PhysX3.0 actor, body and shape.
class PhysX3DescTemplateImpl : public nvidia::PhysX3DescTemplate, public UserAllocated
{
public:
	PhysX3DescTemplateImpl()
	{
		SetToDefault();
	}
	void apply(PxActor* actor) const
	{
		actor->setActorFlags(static_cast<physx::PxActorFlags>(actorFlags));
		actor->setDominanceGroup(dominanceGroup);
		actor->setOwnerClient(ownerClient);
		PX_ASSERT(clientBehaviorBits < UINT8_MAX);
		actor->setClientBehaviorFlags(physx::PxActorClientBehaviorFlags((uint8_t)clientBehaviorBits));
		//actor->contactReportFlags;	// must be set via call PhysX3Interface::setContactReportFlags
		actor->userData	= userData;
		if (name)
		{
			actor->setName(name);
		}

		// body
		PxRigidBody*	rb	= actor->is<physx::PxRigidBody>();
		if (rb)
		{
			// density, user should call updateMassAndInertia when shapes are created.
		}

		PxRigidDynamic*	rd	= actor->is<physx::PxRigidDynamic>();
		if (rd)
		{
			rd->setRigidBodyFlags(physx::PxRigidBodyFlags(bodyFlags));
			rd->setWakeCounter(wakeUpCounter);
			rd->setLinearDamping(linearDamping);
			rd->setAngularDamping(angularDamping);
			rd->setMaxAngularVelocity(maxAngularVelocity);
			// sleepLinearVelocity	attribute for deformable/cloth, see below.
			rd->setSolverIterationCounts(solverIterationCount, velocityIterationCount);
			rd->setContactReportThreshold(contactReportThreshold);
			rd->setSleepThreshold(sleepThreshold);
		}
	}
	void apply(PxShape* shape) const
	{
		shape->setFlags((physx::PxShapeFlags)shapeFlags);
		shape->setMaterials(materials.begin(), static_cast<uint16_t>(materials.size()));
		shape->userData	= shapeUserData;
		if (shapeName)
		{
			shape->setName(shapeName);
		}
		shape->setSimulationFilterData(simulationFilterData);
		shape->setQueryFilterData(queryFilterData);
		shape->setContactOffset(contactOffset);
		shape->setRestOffset(restOffset);
	}

	APEX_ACTOR_TEMPLATE_PARAM(physx::PxDominanceGroup,	DominanceGroup,				dominanceGroup)
	APEX_ACTOR_TEMPLATE_PARAM(uint8_t,				ActorFlags,					actorFlags)
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxClientID,		OwnerClient,				ownerClient)
	APEX_ACTOR_TEMPLATE_PARAM(uint32_t,				ClientBehaviorBits,			clientBehaviorBits)
	APEX_ACTOR_TEMPLATE_PARAM(uint16_t,				ContactReportFlags,			contactReportFlags)
	APEX_ACTOR_TEMPLATE_PARAM(void*,					UserData,					userData)
	APEX_ACTOR_TEMPLATE_PARAM(const char*,				Name,						name)

	APEX_ACTOR_TEMPLATE_PARAM(float,			Density,					density)
	APEX_ACTOR_TEMPLATE_PARAM(uint8_t,				BodyFlags,					bodyFlags)
	APEX_ACTOR_TEMPLATE_PARAM(float,			WakeUpCounter,				wakeUpCounter)
	APEX_ACTOR_TEMPLATE_PARAM(float,			LinearDamping,				linearDamping)
	APEX_ACTOR_TEMPLATE_PARAM(float,			AngularDamping,				angularDamping)
	APEX_ACTOR_TEMPLATE_PARAM(float,			MaxAngularVelocity,			maxAngularVelocity)
	APEX_ACTOR_TEMPLATE_PARAM(float,			SleepLinearVelocity,		sleepLinearVelocity)
	APEX_ACTOR_TEMPLATE_PARAM(uint32_t,				SolverIterationCount,		solverIterationCount)
	APEX_ACTOR_TEMPLATE_PARAM(uint32_t,				VelocityIterationCount,		velocityIterationCount)
	APEX_ACTOR_TEMPLATE_PARAM(float,			ContactReportThreshold,		contactReportThreshold)
	APEX_ACTOR_TEMPLATE_PARAM(float,			SleepThreshold,				sleepLinearVelocity)

	APEX_ACTOR_TEMPLATE_PARAM(uint8_t,				ShapeFlags,					shapeFlags)
	APEX_ACTOR_TEMPLATE_PARAM(void*,					ShapeUserData,				shapeUserData)
	APEX_ACTOR_TEMPLATE_PARAM(const char*,				ShapeName,					shapeName)
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxFilterData,		SimulationFilterData,		simulationFilterData)
	APEX_ACTOR_TEMPLATE_PARAM(physx::PxFilterData,		QueryFilterData,			queryFilterData)
	APEX_ACTOR_TEMPLATE_PARAM(float,			ContactOffset,				contactOffset)
	APEX_ACTOR_TEMPLATE_PARAM(float,			RestOffset,					restOffset)
	physx::PxMaterial**	getMaterials(uint32_t& materialCount) const
	{
		materialCount = materials.size();
		return const_cast<physx::PxMaterial**>(materials.begin());
	}
	bool				setMaterials(physx::PxMaterial** materialArray, uint32_t materialCount)
	{
		const bool valid = materialArray != NULL && materialCount > 0;
		if (!valid)
		{
			materials.reset();
		}
		else
		{
			materials = Array<PxMaterial*>(materialArray, materialArray + materialCount);
		}
		return valid;
	}

	bool isValid()
	{
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(DominanceGroup,			dominanceGroup)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ActorFlags,				actorFlags)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(OwnerClient,				ownerClient)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ClientBehaviorBits,		clientBehaviorBits)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ContactReportFlags,		contactReportFlags)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(UserData,					userData)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(Name,						name)

		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(Density,					density)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(BodyFlags,				bodyFlags)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(WakeUpCounter,			wakeUpCounter)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(LinearDamping,			linearDamping)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(AngularDamping,			angularDamping)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(MaxAngularVelocity,		maxAngularVelocity)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(SleepLinearVelocity,		sleepLinearVelocity)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(SolverIterationCount,		solverIterationCount)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(VelocityIterationCount,	velocityIterationCount)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ContactReportThreshold,	contactReportThreshold)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(SleepThreshold,			sleepLinearVelocity)

		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ShapeFlags,				shapeFlags)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ShapeUserData,			shapeUserData)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ShapeName,				shapeName)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(SimulationFilterData,		simulationFilterData)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(QueryFilterData,			queryFilterData)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(ContactOffset,			contactOffset)
		APEX_ACTOR_TEMPLATE_PARAM_VALID_OR_RETURN(RestOffset,				restOffset)
		if (materials.size() == 0)
		{
			return false;
		}

		return true;
	}

	void SetToDefault()
	{
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(DominanceGroup)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ActorFlags)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(OwnerClient)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ClientBehaviorBits)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ContactReportFlags)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(UserData)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(Name)

		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(Density)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(BodyFlags)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(WakeUpCounter)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(LinearDamping)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(AngularDamping)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(MaxAngularVelocity)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(SleepLinearVelocity)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(SolverIterationCount)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(VelocityIterationCount)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ContactReportThreshold)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(SleepThreshold)

		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ShapeFlags)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ShapeUserData)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ShapeName)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(SimulationFilterData)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(QueryFilterData)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(ContactOffset)
		APEX_ACTOR_TEMPLATE_PARAM_SET_DEFAULT(RestOffset)
		materials.reset();
	}

	void release()
	{
		delete this;
	}

public:
	// actor
	physx::PxDominanceGroup	dominanceGroup;
	uint8_t 				actorFlags;
	physx::PxClientID			ownerClient;
	uint32_t				clientBehaviorBits;
	uint16_t				contactReportFlags;
	void* 				userData;
	const char* 		name;

	// body
	float				density;

	uint8_t				bodyFlags;
	float				wakeUpCounter;
	float				linearDamping;
	float				angularDamping;
	float				maxAngularVelocity;
	float				sleepLinearVelocity;
	uint32_t				solverIterationCount;
	uint32_t				velocityIterationCount;
	float				contactReportThreshold;
	float				sleepThreshold;

	// shape
	uint8_t				shapeFlags;
	Array<PxMaterial*>	materials;
	void*				shapeUserData;
	const char*			shapeName;
	PxFilterData		simulationFilterData;
	PxFilterData		queryFilterData;
	float				contactOffset;
	float				restOffset;
};	// PhysX3DescTemplateImpl

class ApexActorSource
{
public:

	// ActorSource methods

	void setPhysX3Template(const PhysX3DescTemplate* desc)
	{
		physX3Template.set(static_cast<const PhysX3DescTemplateImpl*>(desc));
	}
	bool getPhysX3Template(PhysX3DescTemplate& dest) const
	{
		return physX3Template.get(static_cast<PhysX3DescTemplateImpl&>(dest));
	}
	PhysX3DescTemplate* createPhysX3DescTemplate() const
	{
		return PX_NEW(PhysX3DescTemplateImpl);
	}

	void modifyActor(PxRigidActor* actor) const
	{
		if (physX3Template.isSet)
		{
			physX3Template.data.apply(actor);
		}
	}
	void modifyShape(PxShape* shape) const
	{
		if (physX3Template.isSet)
		{
			physX3Template.data.apply(shape);
		}
	}



protected:

	InitTemplate<PhysX3DescTemplateImpl> physX3Template;
};

#endif // PX_PHYSICS_VERSION_MAJOR == 3

}
} // end namespace nvidia::apex

#endif // __APEX_ACTOR_H__
