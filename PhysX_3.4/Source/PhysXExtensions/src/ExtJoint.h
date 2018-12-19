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


#ifndef NP_JOINTCONSTRAINT_H
#define NP_JOINTCONSTRAINT_H

#include "PsAllocator.h"
#include "PsUtilities.h"
#include "PxConstraint.h"
#include "PxConstraintExt.h"
#include "PxJoint.h"
#include "PxD6Joint.h"
#include "PxRigidDynamic.h"
#include "PxRigidStatic.h"
#include "ExtPvd.h"
#include "PxMetaData.h"
#include "CmRenderOutput.h"
#include "PxPhysics.h"
#include "PsFoundation.h"

#if PX_SUPPORT_PVD
#include "PxScene.h"
#include "PxPvdClient.h"
#include "PxPvdSceneClient.h"
#endif

// PX_SERIALIZATION

namespace physx
{

PxConstraint* resolveConstraintPtr(PxDeserializationContext& v,
								   PxConstraint* old, 
								   PxConstraintConnector* connector,
								   PxConstraintShaderTable& shaders);

// ~PX_SERIALIZATION

namespace Ext
{
	struct JointData
	{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for 
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================
							PxConstraintInvMassScale	invMassScale;
							PxTransform					c2b[2];		
	protected:
		                    ~JointData(){}
	};

	template <class Base, class ValueStruct>
	class Joint : public Base, 
				  public PxConstraintConnector, 
				  public Ps::UserAllocated
	{
  
    public:
// PX_SERIALIZATION
						Joint(PxBaseFlags baseFlags) : Base(baseFlags) {}
		virtual	void	requiresObjects(PxProcessPxBaseCallback& c)
		{			
			c.process(*mPxConstraint);
			
			{
				PxRigidActor* a0 = NULL;
				PxRigidActor* a1 = NULL;
				mPxConstraint->getActors(a0,a1);
				
				if (a0)
				{
					c.process(*a0);
				}
				if (a1)
				{
					c.process(*a1);
				}
			}
		}	
//~PX_SERIALIZATION
		
#if PX_SUPPORT_PVD

		virtual bool updatePvdProperties(physx::pvdsdk::PvdDataStream& pvdConnection, const PxConstraint* c, PxPvdUpdateType::Enum updateType) const
		{
			if(updateType == PxPvdUpdateType::UPDATE_SIM_PROPERTIES)
			{
				Ext::Pvd::simUpdate<Base>(pvdConnection, *this);
				return true;
			}
			else if(updateType == PxPvdUpdateType::UPDATE_ALL_PROPERTIES)
			{
				Ext::Pvd::updatePvdProperties<Base, ValueStruct>(pvdConnection, *this);
				return true;
			}
			else if(updateType == PxPvdUpdateType::CREATE_INSTANCE)
			{
				Ext::Pvd::createPvdInstance<Base>(pvdConnection, *c, *this);
				return true;
			}
			else if(updateType == PxPvdUpdateType::RELEASE_INSTANCE)
			{
				Ext::Pvd::releasePvdInstance(pvdConnection, *c, *this);
				return true;
			}
			return false;
		}
#else
		virtual bool updatePvdProperties(physx::pvdsdk::PvdDataStream&, const PxConstraint*, PxPvdUpdateType::Enum) const
		{
			return false;
		}
#endif



		void getActors(PxRigidActor*& actor0, PxRigidActor*& actor1)	const		
		{	
			if ( mPxConstraint ) mPxConstraint->getActors(actor0,actor1);
			else
			{
				actor0 = NULL;
				actor1 = NULL;
			}
		}
		
		void setActors(PxRigidActor* actor0, PxRigidActor* actor1)
		{	
			//TODO SDK-DEV
			//You can get the debugger stream from the NpScene
			//Ext::Pvd::setActors( stream, this, mPxConstraint, actor0, actor1 );
			PX_CHECK_AND_RETURN(actor0 != actor1, "PxJoint::setActors: actors must be different");
			PX_CHECK_AND_RETURN((actor0 && !actor0->is<PxRigidStatic>()) || (actor1 && !actor1->is<PxRigidStatic>()), "PxJoint::setActors: at least one actor must be non-static");

#if PX_SUPPORT_PVD
			PxScene* scene = getScene();
			if(scene)
			{
				//if pvd not connect data stream is NULL
				physx::pvdsdk::PvdDataStream* conn = scene->getScenePvdClient()->getClientInternal()->getDataStream();
				if( conn != NULL )
					Ext::Pvd::setActors(
					*conn,
					*this,
					*mPxConstraint,
					actor0, 
					actor1
					);
			}
#endif

			mPxConstraint->setActors(actor0, actor1);
			mData->c2b[0] = getCom(actor0).transformInv(mLocalPose[0]);
			mData->c2b[1] = getCom(actor1).transformInv(mLocalPose[1]);
			mPxConstraint->markDirty();
		}

		// this is the local pose relative to the actor, and we store internally the local
		// pose relative to the body 

		PxTransform getLocalPose(PxJointActorIndex::Enum actor) const
		{	
			return mLocalPose[actor];
		}
		
		void setLocalPose(PxJointActorIndex::Enum actor, const PxTransform& pose)
		{
			PX_CHECK_AND_RETURN(pose.isSane(), "PxJoint::setLocalPose: transform is invalid");
			PxTransform p = pose.getNormalized();
			mLocalPose[actor] = p;
			mData->c2b[actor] = getCom(actor).transformInv(p); 
			mPxConstraint->markDirty();
		}

		static	PxTransform	getGlobalPose(const PxRigidActor* actor)
		{
			if(!actor)
				return PxTransform(PxIdentity);
			return actor->getGlobalPose();
		}

		void getActorVelocity(const PxRigidActor* actor, PxVec3& linear, PxVec3& angular) const
		{
			if(!actor || actor->is<PxRigidStatic>())
			{
				linear = angular = PxVec3(0);
				return;
			}
			
			linear = static_cast<const PxRigidBody*>(actor)->getLinearVelocity();
			angular = static_cast<const PxRigidBody*>(actor)->getAngularVelocity();
		}


		PxTransform			getRelativeTransform()					const
		{
			PxRigidActor* actor0, * actor1;
			mPxConstraint->getActors(actor0, actor1);
			const PxTransform t0 = getGlobalPose(actor0) * mLocalPose[0];
			const PxTransform t1 = getGlobalPose(actor1) * mLocalPose[1];
			return t0.transformInv(t1);
		}

		PxVec3	getRelativeLinearVelocity()			const
		{
			PxRigidActor* actor0, * actor1;
			PxVec3 l0, a0, l1, a1;
			mPxConstraint->getActors(actor0, actor1);

			PxTransform t0 = getCom(actor0), t1 = getCom(actor1);
			getActorVelocity(actor0, l0, a0);
			getActorVelocity(actor1, l1, a1);

			PxVec3 p0 = t0.q.rotate(mLocalPose[0].p), 
				   p1 = t1.q.rotate(mLocalPose[1].p);
			return t0.transformInv(l1 - a1.cross(p1) - l0 + a0.cross(p0));
		}

		PxVec3				getRelativeAngularVelocity()		const
		{
			PxRigidActor* actor0, * actor1;
			PxVec3 l0, a0, l1, a1;
			mPxConstraint->getActors(actor0, actor1);

			PxTransform t0 = getCom(actor0);
			getActorVelocity(actor0, l0, a0);
			getActorVelocity(actor1, l1, a1);

			return t0.transformInv(a1 - a0);
		}


		void getBreakForce(PxReal& force, PxReal& torque)	const
		{
			mPxConstraint->getBreakForce(force,torque);
		}

		void setBreakForce(PxReal force, PxReal torque)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(force) && PxIsFinite(torque), "NpJoint::setBreakForce: invalid float");
			mPxConstraint->setBreakForce(force,torque);
		}


		PxConstraintFlags getConstraintFlags()									const
		{
			return mPxConstraint->getFlags();
		}

		void setConstraintFlags(PxConstraintFlags flags)
		{
			mPxConstraint->setFlags(flags);
		}

		void setConstraintFlag(PxConstraintFlag::Enum flag, bool value)
		{
			mPxConstraint->setFlag(flag, value);
		}

		void setInvMassScale0(PxReal invMassScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invMassScale) && invMassScale>=0, "PxJoint::setInvMassScale0: scale must be non-negative");
			mData->invMassScale.linear0 = invMassScale;
			mPxConstraint->markDirty();
		}

		PxReal getInvMassScale0() const
		{
			return mData->invMassScale.linear0;
		}

		void setInvInertiaScale0(PxReal invInertiaScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invInertiaScale) && invInertiaScale>=0, "PxJoint::setInvInertiaScale0: scale must be non-negative");
			mData->invMassScale.angular0 = invInertiaScale;
			mPxConstraint->markDirty();
		}

		PxReal getInvInertiaScale0() const
		{
			return mData->invMassScale.angular0;
		}

		void setInvMassScale1(PxReal invMassScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invMassScale) && invMassScale>=0, "PxJoint::setInvMassScale1: scale must be non-negative");
			mData->invMassScale.linear1 = invMassScale;
			mPxConstraint->markDirty();
		}

		PxReal getInvMassScale1() const
		{
			return mData->invMassScale.linear1;
		}

		void setInvInertiaScale1(PxReal invInertiaScale)
		{
			PX_CHECK_AND_RETURN(PxIsFinite(invInertiaScale) && invInertiaScale>=0, "PxJoint::setInvInertiaScale: scale must be non-negative");
			mData->invMassScale.angular1 = invInertiaScale;
			mPxConstraint->markDirty();
	}

		PxReal getInvInertiaScale1() const
		{
			return mData->invMassScale.angular1;
		}

		const char* getName() const
		{
			return mName;
		}

		void setName(const char* name)
		{
			mName = name;
		}

		void onComShift(PxU32 actor)
		{
			mData->c2b[actor] = getCom(actor).transformInv(mLocalPose[actor]); 
			markDirty();
		}

		void onOriginShift(const PxVec3& shift)
		{
			PxRigidActor* a[2];
			mPxConstraint->getActors(a[0], a[1]);

			if (!a[0])
			{
				mLocalPose[0].p -= shift;
				mData->c2b[0].p -= shift;
				markDirty();
			}
			else if (!a[1])
			{
				mLocalPose[1].p -= shift;
				mData->c2b[1].p -= shift;
				markDirty();
			}
		}

		void* prepareData()
		{
			return mData;
		}

		PxJoint* getPxJoint()
		{
			return this;
		}

		void* getExternalReference(PxU32& typeID)
		{
			typeID = PxConstraintExtIDs::eJOINT;
			return static_cast<PxJoint*>( this );
		}

		PxConstraintConnector* getConnector()
		{
			return this;
		}

		PX_INLINE void setPxConstraint(PxConstraint* pxConstraint)
		{
			mPxConstraint = pxConstraint;
		}

		PX_INLINE PxConstraint* getPxConstraint()
		{
			return mPxConstraint;
		}

		PX_INLINE const PxConstraint* getPxConstraint() const
		{
			return mPxConstraint;
		}

		void release()
		{
			mPxConstraint->release();
		}

		PxBase* getSerializable()
		{
			return this;
		}

		void onConstraintRelease()
		{
			PX_FREE_AND_RESET(mData);
			delete this;
		}

		PxScene* getScene() const
		{
			return mPxConstraint ? mPxConstraint->getScene() : NULL;
		}

	private:
		PxConstraint* getConstraint() const { return mPxConstraint; }

	protected:
		
		PxTransform getCom(PxU32 index) const
		{
			PxRigidActor* a[2];
			mPxConstraint->getActors(a[0],a[1]);
			return getCom(a[index]);
		}

		PxTransform getCom(PxRigidActor* actor) const
		{
			if (!actor)
				return PxTransform(PxIdentity);
			else if (actor->getType() == PxActorType::eRIGID_DYNAMIC || actor->getType() == PxActorType::eARTICULATION_LINK)
				return static_cast<PxRigidBody*>(actor)->getCMassLocalPose();
			else
			{
				PX_ASSERT(actor->getType() == PxActorType::eRIGID_STATIC);
				return static_cast<PxRigidStatic*>(actor)->getGlobalPose().getInverse();
			}
		}

		void initCommonData(JointData& data,
							PxRigidActor* actor0, const PxTransform& localFrame0, 
							PxRigidActor* actor1, const PxTransform& localFrame1)
		{
			mLocalPose[0] = localFrame0.getNormalized();
			mLocalPose[1] = localFrame1.getNormalized();
			data.c2b[0] = getCom(actor0).transformInv(localFrame0);
			data.c2b[1] = getCom(actor1).transformInv(localFrame1);
			data.invMassScale.linear0 = 1.0f;
			data.invMassScale.angular0 = 1.0f;
			data.invMassScale.linear1 = 1.0f;
			data.invMassScale.angular1 = 1.0f;
		}


		Joint(PxType concreteType, PxBaseFlags baseFlags)
		: Base(concreteType, baseFlags)
		, mName(NULL)
		, mPxConstraint(0)
		{
			Base::userData = NULL;
		}


		void markDirty()
		{ 
			mPxConstraint->markDirty();
		}

		virtual const void* getConstantBlock() const { return mData;  }

		const char*						mName;
		PxTransform						mLocalPose[2];
		PxConstraint*					mPxConstraint;
		JointData*						mData;
	};

} // namespace Ext

}

#endif
